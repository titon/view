<?hh // strict
/**
 * @copyright   2010-2014, The Titon Project
 * @license     http://opensource.org/licenses/bsd-license.php
 * @link        http://titon.io
 */

namespace Titon\View;

use Titon\Common\DataMap;
use Titon\Utility\Time;
use Titon\View\Engine;
use Titon\View\Engine\TemplateEngine;

/**
 * Adds support for rendering engines which handle the basics of rendering a template.
 *
 * @package Titon\View\View
 * @events
 *      view.rendering.layout|wrapper|template(View $engine, string $template, Template $type)
 *      view.rendered.layout|wrapper|template(View $engine, string $template, Template $type)
 */
class EngineView extends AbstractView {

    /**
     * Template rendering engine.
     *
     * @type \Titon\View\Engine
     */
    protected Engine $_engine;

    /**
     * Return the rendering engine. If none is set, load the default engine.
     *
     * @return \Titon\View\Engine
     */
    public function getEngine(): Engine {
        if (!$this->_engine) {
            $this->setEngine(new TemplateEngine());
        }

        return $this->_engine;
    }

    /**
     * {@inheritdoc}
     */
    public function render(string $template, bool $private = false): string {
        return $this->cache([__METHOD__, $template, $private], function() use ($template, $private) {
            $this->emit('view.rendering', [$this, &$template]);

            $engine = $this->getEngine();
            $type = $private ? Template::CLOSED : Template::OPEN;

            // Render template
            $this->renderLoop($template, $type);

            // Apply wrappers
            foreach ($engine->getWrappers() as $wrapper) {
                $this->renderLoop($wrapper, Template::WRAPPER);
            }

            // Apply layout
            if ($layout = $engine->getLayout()) {
                $this->renderLoop($layout, Template::LAYOUT);
            }

            $response = $engine->getContent();

            $this->emit('view.rendered', [$this, &$response]);

            return $response;
        });
    }

    /**
     * Rendering of individual template parts for each type: layout, wrapper, etc.
     *
     * @param string $template
     * @param \Titon\View\Template $type
     * @return $this
     */
    public function renderLoop(string $template, Template $type): this {
        $engine = $this->getEngine();

        if ($type === Template::LAYOUT) {
            $event = 'layout';
        } else if ($type === Template::WRAPPER) {
            $event = 'wrapper';
        } else {
            $event = 'template';
        }

        $this->emit('view.rendering.' . $event, [$this, &$template, $type]);

        $engine->setContent($this->renderTemplate(
            $this->locateTemplate($template, $type),
            $this->getVariables()
        ));

        $this->emit('view.rendered.' . $event, [$this, &$template, $type]);

        return $this;
    }

    /**
     * {@inheritdoc}
     */
    public function renderTemplate(string $path, DataMap $variables = Map {}): string {
        $expires = $variables->get('cache');
        $storage = $this->getStorage();
        $key = md5($path);

        if ($expires && $storage) {
            if ($content = $storage->get($key)) {
                return $content;
            }
        }

        $content = $this->getEngine()->render($path, $variables);

        if ($expires && $storage) {
            $storage->set($key, $content, Time::toUnix($expires));
        }

        return $content;
    }

    /**
     * Set the rendering engine.
     *
     * @param \Titon\View\Engine $engine
     * @return $this
     */
    public function setEngine(Engine $engine): this {
        $engine->setView($this);

        $this->_engine = $engine;

        return $this;
    }

}